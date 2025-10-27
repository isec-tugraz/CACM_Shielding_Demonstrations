DOCKER_IMAGE=stefanpranger/cacm_shielding_demonstrations
IMAGE_VERSION=latest
CONTAINER_NAME=tempestpy_experiments_$1

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
--shm-size=4.86gb \
"$DOCKER_IMAGE:$IMAGE_VERSION" \
-c "pip install -e $CONTAINER_MINIGRID_DIR && python $CONTAINER_NOTEBOOK_DIR/$1"
