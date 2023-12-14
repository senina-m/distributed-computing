FROM i386/ubuntu:14.04

RUN sudo apt-get update && sudo apt-get -y install clang-3.5

COPY . /pa4
WORKDIR /pa4


ENV LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/pa4/lib64"
# ENV LD_PRELOAD="/pa4/lib64/libruntime.so"


RUN make
ENTRYPOINT ["./build/pa4", "--mutexl"]