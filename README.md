# libzsync
Peer to Peer file synchronization based on ZeroMQ

## Building zsync
To use or contribute to zsync, build and install this stack:

````
git clone git://github.com/jedisct1/libsodium.git
git clone git://github.com/zeromq/libzmq.git
git clone git://github.com/zeromq/czmq.git
git clone git://github.com/zerosync/libzsync.git
for project in libsodium libzmq czmq libzsync; do
    cd $project
    ./autogen.sh
    ./configure && make check
    sudo make install
    sudo ldconfig
    cd ..
done
````
