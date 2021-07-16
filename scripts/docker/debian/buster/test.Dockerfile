# syntax = docker/dockerfile:1.2
FROM debian:buster

ENV LANG C.UTF-8
ENV LANGUAGE C.UTF-8
ENV LC_ALL C.UTF-8

ARG USERID=1000
RUN adduser elektra --uid ${USERID} \
    && adduser elektra sudo

ENV ELEKTRA_ROOT=/opt/elektra/
RUN mkdir -p ${ELEKTRA_ROOT}

RUN --mount=type=tmpfs,target=/tmp/ajshdkbfasbdjf echo $(mount)
RUN echo $(df -h)

RUN echo "%sudo ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER ${USERID}
WORKDIR /home/elektra

CMD ["/bin/bash","-l"]
