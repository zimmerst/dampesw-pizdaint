FROM zimmerst85/dampesw:v6r0p0
MAINTAINER Stephan Zimmer <zimmer@cern.ch>

# install HEPMC
RUN /bin/bash -c \
    "source /root/setup-externals.sh && \
    cd /tmp/ && \
    curl -o hemc2.06.09.tgz http://hepmc.web.cern.ch/hepmc/releases/hepmc2.06.09.tgz && \
    tar xzvf hemc2.06.09.tgz && \
    cd hepmc2.06.09 && ./bootstrap && \
    mkdir -p /tmp/HepMC/build_dir /opt/HepMC/ && \
    cd /tmp/hepmc2.06.09 && ./configure --prefix=/opt/HepMC/ --with-momentum=MEV --with-length=CM && \
    make -j$(grep -c processor /proc/cpuinfo) && \
    make check && make install"   

COPY setup-externals.sh /root/setup-externals.sh
## adding additional boost stuff
RUN /bin/bash -c \
    "source /root/setup-externals.sh && \
    cd /usr/lib && \
    cd boost_* &&\
    ./bootstrap.sh &&\
    time ./b2 install -j$(grep -c processor /proc/cpuinfo)  \
         --build-type=minimal variant=release \
         --layout=tagged threading=multi \
         --with-python \
         --with-system \
         --with-iostreams \
         --with-program_options \
         --with-filesystem 1>> install.log &&\
    ldconfig"
### adding some silly boost links
RUN echo "creating links" && \
    ln -vs /usr/local/lib/libboost_iostreams-mt.so /usr/local/lib/libboost_iostreams.so && \
    ln -vs /usr/local/lib/libboost_program_options-mt.so /usr/local/lib/libboost_program_options.so

# install CRMC
ADD geant4-crmc.tgz /usr/src/
RUN /bin/bash -c "\
   source /root/setup-externals.sh && \
   mkdir -p /opt/G4CRMC && \
   cd /opt/G4CRMC && \
   cmake --DCMAKE_INSTALL_PREFIX=/opt/G4CRMC /usr/src/crmc && \
   make -j$(grep -c processor /proc/cpuinfo) && \
   make install"

# install DAMPESW 6.0.0
RUN rm -rfv /dampe/releases/DmpSoftware-6-0-0 && mkdir -p /dampe/releases/DmpSoftware-6-0-0
COPY setup-externals.sh /root/setup-externals.sh
ADD DmpSoftware-6-0-0.tgz /tmp/
# apply patches if any
COPY patches /tmp/patches
COPY applyPatch.sh /root/applyPatch.sh
RUN /bin/bash -c \
    "bash /root/applyPatch.sh /tmp/patches /tmp/DmpSoftware-6-0-0" 
RUN /bin/bash -c \
    "source /root/setup-externals.sh && \
    cd /tmp/DmpSoftware-6-0-0 && \
    source pre-install.sh && \
    export DMPSWSYS=/dampe/releases/DmpSoftware-6-0-0 && \
    time scons -j$(grep -c processor /proc/cpuinfo) useCRMC=1"
COPY DmpWorkflow-v2r2p10.tar.gz /tmp/DmpWorkflow-v2r2p10.tar.gz
RUN /bin/bash -c \
    "source /root/setup-externals.sh && \
    pip install /tmp/DmpWorkflow-v2r2p10.tar.gz"
### copy configuration file ###
COPY docker.cfg /opt/conda/lib/python2.7/site-packages/DmpWorkflow/config/settings.cfg


