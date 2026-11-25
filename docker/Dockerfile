# rclsuru を作って試すための環境(ROS 2 Jazzy + clang + Go + suru)
#   docker build -t rclsuru-dev -f docker/Dockerfile .
#   docker run --rm -it -v $PWD:/work/rclsuru -w /work/rclsuru rclsuru-dev
FROM ros:jazzy-ros-base

ARG SURU_VERSION=v0.5.0
ARG GO_VERSION=1.23.4

RUN apt-get update \
 && apt-get install -y --no-install-recommends clang curl ca-certificates python3-pip git \
 && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL "https://go.dev/dl/go${GO_VERSION}.linux-$(dpkg --print-architecture).tar.gz" | tar -C /usr/local -xz
ENV PATH=/root/go/bin:/usr/local/go/bin:${PATH}

# kumu(作る・試験)と suruc、インタプリタ(suru)
RUN go install github.com/suru-lang/suru/cmd/kumu@${SURU_VERSION} \
 && go install github.com/suru-lang/suru/cmd/suruc@${SURU_VERSION} \
 && pip install --break-system-packages "git+https://github.com/suru-lang/suru@${SURU_VERSION}"

RUN echo 'source /opt/ros/jazzy/setup.bash' >> /root/.bashrc
