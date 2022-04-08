FROM daos-dev:rocky8.4
WORKDIR /app
COPY build.sh .
COPY CMakeLists.txt .
COPY src/ src/
COPY lib/ lib/
COPY bench/ bench/
RUN bash ./build.sh
