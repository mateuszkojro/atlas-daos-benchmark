FROM daos-dev:rocky8.4
WORKDIR /app
RUN dnf install -y python3
RUN dnf install -y curl libcurl-devel
COPY metabuild.py .
COPY cmake.in .
COPY CMakeLists.txt .
COPY src/ src/
COPY lib/ lib/
COPY bench/ bench/
RUN ./metabuild.py --configure --build
CMD ["./build/bench/bench"]
