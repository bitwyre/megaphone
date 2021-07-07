# Stage 1 - Build

FROM ubuntu:latest as gcc-builder

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y build-essential automake autoconf autotools-dev make libtool zlib1g-dev && \
    apt-get clean

COPY . /app/megaphone
WORKDIR /app
RUN make deps -C megaphone && \
    make -C megaphone

# Stage 2 - Production Image

FROM ubuntu:latest

LABEL maintainer="Aditya Kresna (kresna@bitwyre.com), Yefta Sutanto (yefta@bitwyre.com)"

COPY --from=gcc-builder /app/megaphone/megaphone /usr/bin/

ENTRYPOINT ["megaphone"]
