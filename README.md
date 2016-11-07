# Dependencies
```
libboost1.58-dev
libgtk2.0-dev
pidgin-dev
```

# Build
```
cd <np1sec-test-client dir>
mkdir -p build
cd build
np1sec_lib_dir=<where libnp1sec.so can be found>
np1sec_inc_dir=<where np1sec headers can be found>
cmake .. -DNP1SEC_LIB_DIR=$np1sec_lib_dir -DNP1SEC_INCLUDE_DIR=$np1sec_inc_dir
make
```

# Prepare for testing
To test it, you'll likely want to run pidgin multiple times on the
same machine. To do so, we'll create one configuration directory for
each participant

```
for i in alice bob charlie; do
  mkdir -p ~/pidgin/$i/plugins
  ln -s <np1sec-test-client dir>/build/libnp1sec-plugin.so ~/pidgin/$i/plugins/
done
```

# Test
For each participant created in the previous step, run

```
pidgin --config=~/pidgin/alice
```

Once started, go to Tools > Plugins and enable the `(n+1)sec Secure messaging" plugin.
