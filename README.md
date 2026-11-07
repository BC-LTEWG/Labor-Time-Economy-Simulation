# Labor Time Economy Agent Based Simulation

Code and supporting documents for the Labor-Time Economy Working Group project at Boston College.

# Usage

## Prerequisites
- A C++17 compiler available as `g++` (the makefile calls `g++`) and GNU `make`.
- Python 3.12 or higher, for the Overseer visualization layer.
- On Windows, `g++` and `make` are not installed by default. Install them separately (for example MinGW-w64 `g++` and GNU `make`) and make sure both are on your `PATH`.

## Building and Running From Command Line
- To build the project, run `make`. To run the simulation, enter `./bin/sim` or `cd bin ; ./sim`.
- To build unit tests, run `make tests`. To run them, enter `cd test ; ./run_tests.sh`. Note that you cannot run the `run_tests.sh` script from outside of `test` because it uses a glob expression that depends on the directory from which the script is run.

## Building and Running Through Overseer
The LTE comes with a robust visualization layer for [Overseer](https://github.com/alexbcreiner0/Overseer), allowing one to explore the model interactively, experimenting with different parameter settings and viewing evolution in real time. We also provide a script which can get you started with this layer very quickly. 
1. Open up a terminal and clone the repo: `git clone --recurse-submodules https://github.com/BC-LTEWG/Labor-Time-Economy-Simulation` **Important**: Use the `--recurse-submodules` flag when you clone it, so as to also clone Overseer's source code:
```
git clone --recurse-submodules https://github.com/BC-LTEWG/Labor-Time-Economy-Simulation
```
If you have already cloned the repo and did not use this flag, you can either clone Overseer into the repo's root directory manually, or, equivalently, run the following command from inside of it:
```
git submodule update --init --recursive
```
2. Navigate inside of the repo and run one of the following two scripts depending on your operating system: 
- If you are on Mac OS or Linux, run `./setup.sh`. If you are a Mac OS user, you may first need to make the script executable before this command will work: `chmod +x setup.sh`
- If you are on Windows, run `.\setup.ps1` (or simply right click the file and choose Run with PowerShell). (Note that you must use PowerShell to run this script. Do not try to use `cmd`.)
3. In either case, this script will create a file called `run.sh` or `run.ps1`, again depending on your operating system. Running that file in the same way should launch Overseer and immediately begin the simulation. 

If the setup script fails for any reason and you address the source of the failure, you should be able to run the script again without any issue. You can also do this if you accidentally delete your `run.sh` file, which is created by the script.

For more information on using Overseer, see [here](https://overseer-modeling.readthedocs.io/en/latest/). 

Additional notes:
- Overseer comes with a variety of other detailed economic models, which are *not* included when you install Overseer through this `setup.sh` path. To access these, simply go to your Application Settings in Overseer and change your user models directory to anything else, making sure that the leaf folder does not exist yet. For example, if you want all of your models to appear inside of your user Documents on Linux, you would write in `home/<username>/Documents/Overseer/models`, and make sure that there is no models directory inside of your `~/Documents/Overseer` folder yet. The next time you launch Overseer, it will detect that this folder is missing, create it, and dump all of the prepackaged models inside of it. 
- If you want the Labor Time Economy simulation to appear in your available demos list after doing this, simply drag the `overseer_model` directory into that folder which you created.
