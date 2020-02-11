# Stage 1 - Build

FROM ubuntu:18.04 as gcc-builder

LABEL maintainer.0="Aditya Kresna (kresna@bitwyre.com)"

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y build-essential automake autoconf autotools-dev make libtool zlib1g-dev && \
    apt-get clean

COPY . /app/megaphone
WORKDIR /app
RUN make deps -C megaphone && make -C megaphone

# Stage 2 - Production Image

FROM ubuntu:18.04

COPY --from=gcc-builder /app/megaphone/megaphone /usr/bin/

ENTRYPOINT ["megaphone"]
