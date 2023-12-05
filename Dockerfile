# build time subroutines
FROM ubuntu:latest AS build
ENV DEBIAN_FRONTEND=noninteractive

# Install the compiler and the needed libs/utils
RUN apt update && apt -y install gcc g++ cmake git zlib1g-dev curl

# Install Rust for Zenoh
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="/root/.cargo/bin:${PATH}"

# Build
WORKDIR /megaphone
COPY ./ /megaphone

WORKDIR /megaphone/build
RUN cmake ..
RUN make -j`nproc`

# runtime subroutines
FROM ubuntu:latest

WORKDIR /megaphone
COPY --from=build /megaphone/build/megaphone /usr/local/bin/megaphone
COPY --from=build /megaphone/zenohpico.json5 /usr/local/bin/zenohpico.json5
COPY ./entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod +x /usr/local/bin/megaphone /usr/local/bin/entrypoint.sh
RUN echo ". /vault/secrets/config" > /root/.bashrc

ENTRYPOINT ["entrypoint.sh"]
