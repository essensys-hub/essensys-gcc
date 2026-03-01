# ============================================================
# essensys-builder: Multi-toolchain Docker image
#
# Toolchains included:
#   1. m68k-elf-gcc 14.2 — ColdFire MCF52259 bare-metal (via crosstool-NG 1.26.0)
#   2. Microchip XC8       — PIC16F946 (mode Free, installeur non redistribuable)
#   3. GCC host            — Tests unitaires et integration
#
# Usage:
#   docker build -t essensys-builder .
#   docker run --rm essensys-builder m68k-elf-gcc --version
#   docker run --rm -v $(pwd):/workspace essensys-builder build.sh all
#
# ============================================================

# ------------------------------------------------------------------
# Stage 1: Build m68k-elf-gcc toolchain via crosstool-NG
# ------------------------------------------------------------------
FROM debian:bookworm-slim AS toolchain-builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    autoconf automake libtool libtool-bin \
    texinfo help2man \
    gawk bison flex \
    git wget curl \
    python3 python3-dev \
    libncurses-dev \
    unzip xz-utils \
    file \
    ca-certificates \
    rsync \
    && rm -rf /var/lib/apt/lists/*

# Build crosstool-NG 1.26.0 from source
WORKDIR /tmp/ct-ng-src
RUN git clone --depth 1 --branch crosstool-ng-1.26.0 \
    https://github.com/crosstool-ng/crosstool-ng.git . && \
    ./bootstrap && \
    ./configure --prefix=/opt/ct-ng && \
    make -j"$(nproc)" && \
    make install

ENV PATH="/opt/ct-ng/bin:${PATH}"

# Build m68k-elf cross-toolchain (GCC 14.2.0, binutils 2.43, newlib 4.5, GDB 15.2)
RUN mkdir -p /tmp/src
WORKDIR /tmp/toolchain-build
COPY ct-ng.config .config
RUN ct-ng olddefconfig && ct-ng build

# ------------------------------------------------------------------
# Stage 2: Final image with all toolchains
# ------------------------------------------------------------------
FROM debian:bookworm-slim AS essensys-builder

LABEL maintainer="essensys-hub"
LABEL description="Multi-toolchain build environment for Essensys firmware (ColdFire MCF52259 + PIC16F946)"

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    make \
    srecord \
    git \
    python3 \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

# --- m68k-elf-gcc (ColdFire V2 MCF52259) ---
# Copied from the toolchain-builder stage to keep the final image small.
# Generates ColdFire V2 code with: m68k-elf-gcc -mcpu=52259 -msoft-float
COPY --from=toolchain-builder /opt/x-tools/m68k-elf /opt/x-tools/m68k-elf
ENV PATH="/opt/x-tools/m68k-elf/bin:${PATH}"

# --- Microchip XC8 compiler (PIC16F946, mode Free) ---
# ============================================================
# L'installeur XC8 est téléchargé dynamiquement lors du build docker.
#
# Sans XC8 (build BP uniquement) :
#   docker build --build-arg SKIP_XC8=1 -t essensys-builder .
# ============================================================
ARG SKIP_XC8=0
ARG XC8_URL=https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc8-v3.10-full-install-linux-x64-installer.run
RUN if [ "$SKIP_XC8" = "0" ]; then \
    echo "Downloading XC8..." && \
    wget -q ${XC8_URL} -O /tmp/xc8_installer.run && \
    chmod +x /tmp/xc8_installer.run && \
    /tmp/xc8_installer.run \
    --mode unattended \
    --unattendedmodeui none \
    --netservername "" \
    --LicenseType FreeMode \
    --prefix /opt/microchip/xc8 && \
    rm -f /tmp/xc8_installer.run; \
    else \
    echo "XC8 skipped (SKIP_XC8=$SKIP_XC8)"; \
    fi
ENV PATH="/opt/microchip/xc8/bin:${PATH}"

# --- Build orchestrator ---
COPY build.sh /usr/local/bin/build.sh
RUN chmod +x /usr/local/bin/build.sh

WORKDIR /workspace
ENTRYPOINT []
CMD ["bash"]
