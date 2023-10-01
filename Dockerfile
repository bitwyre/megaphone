# Stage 1 - Build

FROM ubuntu:20.04 as gcc-builder

RUN apt-get update && \
    apt-get upgrade -y && \
    apt-get install -y build-essential automake autoconf autotools-dev make libtool zlib1g-dev && \
    apt-get clean

COPY shared/megaphone/ /app/megaphone

WORKDIR /app
RUN make deps -C megaphone && \
    make -C megaphone

# Stage 2 - Production Image

FROM ubuntu:20.04 

COPY --from=gcc-builder /app/megaphone/megaphone /usr/bin/
COPY --from=gcc-builder /app/megaphone/entrypoint.sh /usr/bin/entrypoint.sh

RUN chmod +x /usr/bin/megaphone /usr/bin/entrypoint.sh

ENTRYPOINT ["entrypoint.sh"]