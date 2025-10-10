DOCKER_IMAGE=cacm_shielding_framework
IMAGE_VERSION=latest
CONTAINER_NAME=tempestpy_jupyter

NOTEBOOK_DIR=./notebooks
CONTAINER_NOTEBOOK_DIR=/opt/notebooks
LOGRESULTS_DIR=./logresults
CONTAINER_LOGRESULTS_DIR=/opt/logresults
MINIGRID_DIR=./notebooks/environments/Minigrid
CONTAINER_MINIGRID_DIR=/opt/Minigrid

sudo docker run \
--name "$CONTAINER_NAME" \
--mount "type=bind,src=$NOTEBOOK_DIR,dst=$CONTAINER_NOTEBOOK_DIR" \
--mount "type=bind,src=$LOGRESULTS_DIR,dst=$CONTAINER_LOGRESULTS_DIR" \
--mount "type=bind,src=$MINIGRID_DIR,dst=$CONTAINER_MINIGRID_DIR" \
--rm \
-p "6006:6006" \
-p "8888:8888" \
--shm-size=4.86gb \
--entrypoint sh \
"$DOCKER_IMAGE:$IMAGE_VERSION" \
-c "pip install -e $CONTAINER_MINIGRID_DIR && jupyter lab --ip=0.0.0.0 --allow-root --notebook-dir=$CONTAINER_NOTEBOOK_DIR --no-browser"
