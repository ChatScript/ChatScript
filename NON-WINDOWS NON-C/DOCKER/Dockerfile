FROM ubuntu:18.04

# Install system packages/dependencies
RUN apt-get --yes update \
 && apt-get --no-install-recommends --yes install libcurl3 \
 && rm -rf /var/lib/apt/lists/*

# Copy minimal set of ChatScript files
COPY ./BINARIES/LinuxChatScript64 /opt/ChatScript/BINARIES/
COPY ./DICT/ENGLISH/*.bin /opt/ChatScript/DICT/ENGLISH/
COPY ./LIVEDATA/SYSTEM /opt/ChatScript/LIVEDATA/SYSTEM
COPY ./LIVEDATA/ENGLISH /opt/ChatScript/LIVEDATA/ENGLISH
COPY ./TOPIC/BUILD0 /opt/ChatScript/TOPIC/BUILD0

# Make binary executable
RUN chmod +x /opt/ChatScript/BINARIES/LinuxChatScript64

# Create entrypoint script so we can pass args and Ctrl+C out
RUN { \
echo '#!/bin/bash'; \
echo 'set -e'; \
echo '/opt/ChatScript/BINARIES/LinuxChatScript64 $@'; \
} > /entrypoint-chatscript.sh \
&& chmod +x /entrypoint-chatscript.sh

# Declare volumes for mount point directories
VOLUME ["/opt/ChatScript/USERS/", "/opt/ChatScript/LOGS/"]

# Set runtime properties
ENV LANG C.UTF-8
EXPOSE 1024
WORKDIR /opt/ChatScript/BINARIES
ENTRYPOINT ["/entrypoint-chatscript.sh"]