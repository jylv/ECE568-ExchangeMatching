FROM ubuntu:20.04
ENV DEBIAN_FRONTEND noninteractive
RUN apt update
RUN apt install -y g++ make
RUN apt -y install postgresql postgresql-client
RUN apt install -y libpqxx-dev libtbb-dev libboost-all-dev
RUN mkdir /stock-exchange-server
WORKDIR /stock-exchange-server
ADD . /stock-exchange-server/
RUN cd /stock-exchange-server
RUN make clean
RUN make