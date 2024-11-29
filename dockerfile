FROM movesrwth/storm-basesystem:ubuntu-22.04 as build

RUN apt-get update
RUN apt-get install -y python3.10

# Specify number of threads to use for parallel compilation
# This number can be set from the commandline with:
# --build-arg no_threads=<value>
ARG no_threads=6

# Build carl
RUN git clone -b c++14-22.01 https://github.com/ths-rwth/carl.git /opt/carl
RUN mkdir -p /opt/carl/build
WORKDIR /opt/carl/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release  -DUSE_CLN_NUMBERS=ON -DUSE_GINAC=ON
RUN make lib_carl -j $no_threads

# Build tempest
RUN git clone -b tempestpy_adaptions https://git.pranger.xyz/sp/tempest.git /opt/tempest
RUN mkdir -p /opt/tempest/build
WORKDIR /opt/tempest/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DSTORM_DEVELOPER=OFF -DSTORM_LOG_DISABLE_DEBUG=ON -DSTORM_PORTABLE=OFF -DSTORM_USE_SPOT_SHIPPED=ON
RUN make resources storm binaries -j $no_threads

# Configure carl-parser
RUN cmake .. -DCMAKE_BUILD_TYPE=$build_type

# Manage python packages
WORKDIR /opt/
RUN apt-get update --fix-missing
RUN apt-get install -y python3-venv


# Python env handling
ENV VIRTUAL_ENV=/opt/venv
RUN python3 -m venv $VIRTUAL_ENV
ENV PATH="$VIRTUAL_ENV/bin:$PATH"

RUN apt-get install -y python3-pip


# build pycarl
RUN git clone -b 2.0.4 https://github.com/moves-rwth/pycarl.git /opt/pycarl
WORKDIR /opt/pycarl
RUN python setup.py build_ext -j $no_threads
RUN pip install .

# build tempestpy
RUN git clone -b refactoring https://git.pranger.xyz/sp/tempestpy.git /opt/tempestpy
WORKDIR /opt/tempestpy
RUN python3 setup.py build_ext --storm-dir /opt/tempest/ -j $no_threads develop

# build yaml-cpp
COPY ./yaml-cpp /opt/yaml-cpp
RUN mkdir -p /opt/yaml-cpp/build
WORKDIR /opt/yaml-cpp/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release
RUN make -j $no_threads
RUN make install -j $no_threads

COPY ./Minigrid2PRISM /opt/Minigrid2PRISM

# build minigrid to prism
RUN mkdir -p /opt/Minigrid2PRISM/build
WORKDIR /opt/Minigrid2PRISM/build
RUN cmake ..
RUN make -j $no_threads


RUN apt-get update && apt-get install ffmpeg libsm6 libxext6 -y

WORKDIR /opt/tempestpy
RUN pip install dm-tree
RUN pip install opencv-python
RUN pip install scikit-image
RUN pip install torch
RUN pip install tensorboard
RUN pip install tensorboardX
RUN pip install tensorflow
RUN pip install jupyterlab
RUN pip install astar
RUN pip install ipywidgets
RUN pip install matplotlib
RUN pip install sb3-contrib
RUN pip install opencv-python
RUN pip install moviepy
RUN pip install gymnasium==0.29.0
RUN pip install numpy==1.24.4

ENV M2P_BINARY=/opt/Minigrid2PRISM/build/main
RUN apt-get install bash -y

ENTRYPOINT ["/bin/bash"]
