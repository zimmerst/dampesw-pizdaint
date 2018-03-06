FROM zimmerst85/dampestack-ext
MAINTAINER Stephan Zimmer <zimmer@cern.ch>

# install DAMPESW 6.0.0
RUN mkdir -p /dampe/releases/DmpSoftware-6-0-0
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
    time scons -j$(grep -c processor /proc/cpuinfo)"
COPY DmpWorkflow-v2r2p10.tar.gz /tmp/DmpWorkflow-v2r2p10.tar.gz
RUN /bin/bash -c \
    "source /root/setup-externals.sh && \
    pip install /tmp/DmpWorkflow-v2r2p10.tar.gz"
### copy configuration file ###
COPY docker.cfg /opt/conda/lib/python2.7/site-packages/DmpWorkflow/config/settings.cfg


