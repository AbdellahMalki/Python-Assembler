FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends wget ca-certificates

RUN wget https://www.uvolante.org/apt/uvolante.sources -O /etc/apt/sources.list.d/uvolante.sources
RUN sh -c 'echo "deb [arch=amd64] http://archive.debian.org/debian/ bullseye contrib main non-free" > /etc/apt/sources.list.d/debian-bullseye.list'
RUN apt-get update && apt-get upgrade -y

RUN apt-get install -y --no-install-recommends \
    gdb \
    build-essential \
    make \
    astyle  \
    indent \
    valgrind \
    gcc \
    clang \
    clang-tidy \
    clangd \
    git \
    python2.7 \
    pyc-objdump

CMD ["/bin/bash"]