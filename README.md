To test this bug in RR:

First build:
```bash
chmod +x build.sh
./build.sh
```

Then try without rr:

```bash
  ./tracer
```

Then with RR (it will hang indefintely)

```bash
  rr record ./tracer
```