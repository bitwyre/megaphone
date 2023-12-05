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
COPY --from=build /megaphone/build/ /usr/local/bin/
COPY --from=build /megaphone/build/_deps/zenohc_backend-build/release/target/release/libzenohc.so /usr/local/bin/libzenohc.so
COPY --from=build /megaphone/zenohpico.json5 /usr/local/bin/zenohpico.json5
COPY ./entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod +x /usr/local/bin/megaphone /usr/local/bin/entrypoint.sh

ENV MEGAPHONE_UWS_PASSPHRASE=12345
# TODO: Find a better way to do this.
ENV MEGAPHONE_SUPPORTED_INSTRUMENTS=ada_usdt_spot,ada_usdx_futures_perp,algo_usdx_futures_perp,arb_usdx_futures_perp,audf_jidr_spot,avax_usdt_spot,avax_usdx_futures_perp,bidr_idr_spot,bnb_usdt_spot,bnb_usdx_futures_perp,btc_usdt_spot,btc_usdt_spot,btc_usdt_spot,btc_usdt_spot,btc_usdt_spot,btc_usdx_futures_perp,btc_usdx_futures_perp,btc_usdx_futures_perp,btc_usdx_futures_perp,btc_usdx_futures_perp,busd_usd_spot,cnli_usd_spot,crv_usdx_futures_perp,doge_usdx_futures_perp,dot_usdx_futures_perp

EXPOSE 8080
ENTRYPOINT ["entrypoint.sh"]
