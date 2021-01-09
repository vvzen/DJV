export DJV_BUILD=$PWD
export PATH=$DJV_BUILD/DJV-install-Debug/bin:$PATH
export DYLD_LIBRARY_PATH=$DJV_BUILD/DJV-install-Debug/lib:$DYLD_LIBRARY_PATH

# Build and install DJV third party
if [ ! -d DJV-third-party-Debug ]; then
	mkdir DJV-third-party-Debug
fi
cd DJV-third-party-Debug
cmake ../third-party -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$DJV_BUILD/DJV-install-Debug -DCMAKE_FIND_FRAMEWORK="LAST"
cmake --build . -j 8
cmake --build . -j 8 --target install
cd ..

# Build and install DJV
if [ ! -d DJV-Debug ]; then
	mkdir DJV-Debug
fi
cd DJV-Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$DJV_BUILD/DJV-install-Debug -DCMAKE_PREFIX_PATH=$DJV_BUILD/DJV-install-Debug -DCMAKE_FIND_FRAMEWORK="LAST"
cmake --build . -j 8

# Run tests
cmake --build . -j 8 --target test

