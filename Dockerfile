FROM murachue/toolchain64:o32tools
RUN apt-get update && apt-get install -y imagemagick ruby sox && apt-get clean
