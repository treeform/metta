FROM mettaai/metta-base:latest

RUN pip install boto3 chex hydra-core matplotlib pettingzoo
RUN pip install rich scipy tabulate

# Use ARG to create a build-time variable
ARG CACHE_DATE

# Use the ARG in a RUN command to break cache
RUN echo "Cache breaker: ${CACHE_DATE}"

# Checkout metta and submodules
WORKDIR /workspace/
RUN git clone --recursive https://github.com/Metta-AI/metta.git metta
WORKDIR /workspace/metta

RUN pip install -r requirements.txt
RUN make install

WORKDIR /workspace/metta/
ENTRYPOINT ["/bin/bash", "-c"]
