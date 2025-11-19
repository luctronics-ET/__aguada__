FROM postgis/postgis:15-3.4

# Install TimescaleDB package on top of PostGIS image
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends wget gnupg lsb-release ca-certificates \
 && wget -qO- https://packagecloud.io/timescale/timescaledb/gpgkey | apt-key add - \
 && echo "deb https://packagecloud.io/timescale/timescaledb/ubuntu $(lsb_release -c -s) main" \
      > /etc/apt/sources.list.d/timescaledb.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends timescaledb-2-postgresql-15 \
 && rm -rf /var/lib/apt/lists/*

# Use the original image entrypoint/command
CMD ["docker-entrypoint.sh"]
