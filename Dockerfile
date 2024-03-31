# Use the Ubuntu base image
FROM ubuntu

# Set the working directory inside the container
WORKDIR /home/project

# Install GCC and Git using apt-get
RUN apt-get update && \
    apt-get install -y gcc git protobuf-c-compiler protobuf-compiler libprotobuf-c-dev

# Set up a bind mount from the host to the guest
VOLUME [".:/home/project"]

# Set the main process to "tail -f /dev/null"
CMD ["tail", "-f", "/dev/null"]
