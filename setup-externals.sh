#!/bin/bash
wd=$(pwd)
source /opt/rh/devtoolset-4/enable
echo "gcc: $(which gcc)"

export PATH=/opt/conda/bin:$PATH
export LD_LIBRARY_PATH=/opt/conda/lib:${LD_LIBRARY_PATH}
export LIBRARY_PATH=/opt/conda/lib:${LIBRARY_PATH}
echo "python: $(which python)"

cd /opt/root*
source bin/thisroot.sh
echo "ROOT: $(which root)"

#cd /opt/geant-*/share/Geant*/geant4make
export G4FORCENUMBEROFTHREADS=2
cd /opt/geant-4.10.03.p02/share/Geant4-10.3.2/geant4make
source geant4make.sh
echo "G4INSTALL: $G4INSTALL"
echo "G4LIB:     $G4LIB"
cd ${wd}

# adding HepMC
export HEPMC_PREFIX=/opt/HepMC

# CRMC
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/opt/G4CRMC/lib
export DYLD_LIBRARY_PATH=${DYLD_LIBRARY_PATH}:/opt/G4CRMC/lib
export CPATH=${CPATH}:/opt/G4CRMC/src
export CPATH=${CPATH}:/usr/src/crmc/src
export CRMC_CONFIG_FILE=/opt/G4CRMC/crmc.param
