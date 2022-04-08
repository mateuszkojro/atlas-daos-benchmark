FROM rockylinux/rockylinux:8.4
RUN dnf upgrade -y
RUN dnf install -y git python3-pip
RUN git clone --recursive https://github.com/daos-stack/daos/
WORKDIR /daos
RUN git checkout v2.0.2
RUN dnf install -y dnf-plugins-core
RUN dnf install -y epel-release
RUN dnf config-manager --set-enabled powertools
RUN dnf update -y
COPY ./daos-patches ./daos-patches
RUN git config --global user.email "you@example.com"
RUN git config --global user.name "Your Name"
RUN git apply ./daos-patches/dnf_install_dependencies_auto_confirm.patch
RUN bash utils/scripts/install-centos8.sh
RUN dnf install -y python3-pyelftools
RUN scons-3 install -j$(nproc --ignore=2) --build-deps=yes --config=force

