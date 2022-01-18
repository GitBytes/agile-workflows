FROM ubuntu:hirsute
MAINTAINER Marco Minutoli <marco.minutoli@pnnl.gov>

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get install -y openssh-server sudo build-essential libopenmpi-dev git \
    wget cmake libgoogle-perftools-dev libgtest-dev python3-pip && \
    pip install --trusted-host pypi.org --trusted-host pypi.python.org --trusted-host files.pythonhosted.org conan

RUN mkdir /var/run/sshd
RUN echo 'root:agile' | chpasswd
RUN echo 'PermitRootLogin yes' > /etc/ssh/sshd_config.d/root_login.conf

# Add a user for the tutorial
RUN adduser --disabled-password --gecos "" agile && \
    echo "agile ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
ENV HOME /home/agile

# Setup Keys
RUN mkdir /home/agile/.ssh
ADD ssh/config /home/agile/.ssh/config
ADD ssh/id_rsa.mpi /home/agile/.ssh/id_rsa
ADD ssh/id_rsa.mpi.pub /home/agile/.ssh/id_rsa.pub
ADD ssh/id_rsa.mpi.pub /home/agile/.ssh/authorized_keys


# Install GMT
WORKDIR /home/agile

RUN chmod -R 600 /home/agile/.ssh/* && \
    chown -R agile:agile /home/agile/.ssh

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D"]
