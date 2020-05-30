Clay Antor has contributed a docker ability for CS.

Building

Building the image is as easy as cloning the repo and building using the Dockerfile.

$ git clone https://github.com/claytantor/chatscript-docker.git
$ cd chatscript-docker
$ docker build -t claytantor/chatscript-docker:latest .
Running The Container
You will need to run the image with the -t option because the chatscript software needs
a tty to work.

docker run -t -d --name chatscript -p 1024:1024 claytantor/chatscript-docker:latest

Running The Client
A simple client script is installed on the docker image to make it convenient to test that the
application is running.

docker exec -it chatscript /bin/chatscript-client


Docker Pull Command

docker pull claytantor/chatscript-docker


-----------------------------------------------

## Docker image

### Building the base Docker image

The `Dockerfile` in this repository provides a ChatScript server with no bots. To build and run it, run the following commands:

```
docker build -t chatscript .
docker run -it -p 1024:1024 chatscript
```

Note: You will probably want to replace the image tag `chatscript` with a more meaningful one for your purposes.

### Building a Docker image containing bot data

Adding bot data to the base image above is as simple as writing a `Dockerfile` like the following one, which builds the `Harry` bot:

```
FROM chatscript

# Copy raw data needed for Harry
COPY ./RAWDATA/filesHarry.txt
COPY ./RAWDATA/HARRY /opt/ChatScript/RAWDATA/HARRY
COPY ./RAWDATA/QUIBBLE /opt/ChatScript/RAWDATA/QUIBBLE

# Build Harry
RUN /opt/ChatScript/BINARIES/LinuxChatScript64 local build1=filesHarry.txt
```

This `Dockerfile` can then be built and run in the same manner as the base `chatscript` image:

```
docker build -t chatscript-harry .
docker run -it chatscript-harry local
```