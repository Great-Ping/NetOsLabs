FROM gcc:14.2
RUN apt-get update
RUN apt-get install -y gdb

COPY ./src /netos-lab/src
WORKDIR /netos-lab/src

RUN gcc -g -o app.exe main.c
# CMD ["./app.exe"]