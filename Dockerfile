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
ENV MEGAPHONE_SUPPORTED_INSTRUMENTS=eth_usdt_spot,xmr_usdt_spot,zec_usdt_spot,btc_usdc_spot,eth_usdc_spot,xmr_usdc_spot,zec_usdc_spot,usdc_idr_spot,usdc_usd_spot,usdt_idr_spot,usdt_usd_spot,btc_usd_spot,eth_usd_spot,xmr_usd_spot,zec_usd_spot,btc_idr_spot,eth_idr_spot,xmr_idr_spot,zec_idr_spot,xch_usdt_spot,atom_usdt_spot,rose_usdt_spot,xtz_usdt_spot,usdt_brl_spot,usdt_mxn_spot,usdc_jidr_spot,usdt_jidr_spot,jidr_idr_spot,busd_idr_spot,ada_idr_spot,bnb_idr_spot,sol_idr_spot,avax_idr_spot,dai_idr_spot,trx_idr_spot,busd_usd_spot,ada_usdt_spot,bnb_usd_spot,avax_usdt_spot,dai_usd_spot,trx_usdt_spot,xtz_usd_spot,btc_jidr_spot,eth_jidr_spot,xmr_jidr_spot,zec_jidr_spot,busd_jidr_spot,ada_jidr_spot,bnb_jidr_spot,sol_jidr_spot,avax_jidr_spot,dai_jidr_spot,trx_jidr_spot,tcota_usd_spot,cnli_usd_spot,mchna_usd_spot,ada_usdt_spot,avax_usdt_spot,bnb_usdt_spot,busd_usdt_spot,sol_usdt_spot,trx_usdt_spot,usdc_usd_spot,usdt_usd_spot,jidr_idr_spot,btc_usdx_futures_perp,eth_usdx_futures_perp,xrp_usdx_futures_perp,ltc_usdx_futures_perp,trx_usdx_futures_perp,ada_usdx_futures_perp,xmr_usdx_futures_perp,zec_usdx_futures_perp,bnb_usdx_futures_perp,algo_usdx_futures_perp,doge_usdx_futures_perp,dot_usdx_futures_perp,crv_usdx_futures_perp,sol_usdx_futures_perp,avax_usdx_futures_perp,matic_usdx_futures_perp,op_usdx_futures_perp,ldo_usdx_futures_perp,arb_usdx_futures_perp,pepe_usdt_spot,pepe_usdx_futures_perp,idk_idr_spot,idrt_idr_spot,near_usdt_spot,btc_usdt_spot,usdt_audf_spot,audf_jidr_spot,bnb_jidr_spot,sol_jidr_spot,trx_jidr_spot,btc_jidr_spot,eth_jidr_spot,dai_jidr_spot,busd_usd_spot,fdusd_usd_spot
EXPOSE 8080
ENTRYPOINT ["entrypoint.sh"]
