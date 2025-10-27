# Shields for Safe Reinforcement Learning

This is the artifact repository for the CACM contributed article "Shields for Safe Reinforcement Learning"

You can find the online article here: https://dl.acm.org/doi/10.1145/3715958

## Shielding Demonstrations

This artifact contains the necessary code to reproduce the experimental results presented in the paper. The artifact uses docker to provide an easy way to reproduce the results.

## Installing the artifact

You can download the image from the docker registry, by running:

```
 docker pull stefanpranger/cacm_shielding_demonstrations
```

This will make the image `stefanpranger/cacm_shielding_demonstrations` available on your machine.

Optionally, you can build the image by running:

```
./docker_build.sh
```
After this, an image with the tag `cacm_shielding_demonstrations` should be available on your machine. If you choose this option, you need to change `DOCKER_IMAGE` in `docker_run_experiments.sh` and `docker_run_jupyter.sh` to `cacm_shielding_demonstrations`.

## Running the Experiments

You can run the experiments presented in the paper via

```
./docker_run_experiments.sh <script>.py
```

with `<script>` being one of:

```
DeterministicShielding
DeterministicUnshielded
ProbabilisticShielding_0.0
ProbabilisticShielding_0.03
ProbabilisticShielding_0.05
ProbabilisticUnshielded
```

`DeterministicShielding` and `DeterministicUnshielded` perform one run of RL training in the deterministic environment, with and without the shield respectively.

`ProbabilisticShielding_<threshold>` performs one run of RL training in the probabilistic environment with the given `<threshold>` for the shield. `ProbabilisticUnshielded` performs an unshielded run of RL training.

You can find the training results in `./logresults`, plotted as individual graphs.

## Running the Container with Jupyter

You can also start the container with:

```
./docker_run_jupyter.sh
```

This allows you to access jupyter notebooks via your browser. The output of the running docker container will contain a link to the jupyter lab. It will have the following format:

```
 http://127.0.0.1:8888/lab?token=<token>
```

The jupyter lab contains two notebooks, namely

- `DeterministicShielding.ipynb` and
- `ProbabilisticShielding.ipynb`.

In the notebooks, you may adapt `value_for_training` and `shielding` to adapt the parameters for training.

