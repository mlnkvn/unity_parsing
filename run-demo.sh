#!/bin/bash

docker build -f demo/Dockerfile . -t unity_parser:demo

docker run -it unity_parser:demo

