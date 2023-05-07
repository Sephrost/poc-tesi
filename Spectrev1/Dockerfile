FROM debian:latest
# intall gcc and make
RUN apt update && apt install -y gcc make git

# To build and mount this folder to the container
# docker build --tag spectre:latest . 
# docker run -it -v $(pwd):/spectre spectre:latest