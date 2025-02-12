FROM gcc:14.2
COPY ./src /netos-lab/src
WORKDIR /netos-lab/src
RUN gcc -o app.exe main.c
CMD ["./app.exe"]