FROM ubuntu:latest

RUN apt-get update && apt-get install -y g++
RUN apt-get install -y libyaml-cpp-dev
RUN apt-get install -y cmake

WORKDIR /app

COPY tool.cpp .
COPY demo/CMakeLists.txt .
COPY demo/run-and-dump.sh .
COPY TestCase01 TestCase01

CMD ["/app/run-and-dump.sh"]
