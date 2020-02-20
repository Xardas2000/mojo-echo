# Mojo echo server + workers

To run it, you need to:

1. Download chromium sources https://chromium.googlesource.com/chromium/src/+/master/docs/linux/build_instructions.md
2. Build it
3. ...
4. Profit

Joke!

Add echo server dependency to chromium src/BUILD.gn

```
group("gn_all") {
  ...
  deps += [
    "//etc/examples/mojo:echo_server",
    "//etc/examples/mojo:echo_worker",
  ]
  ...
}
```


Build echo server

```
ninja -C out/Debug echo_server
```

```
ninja -C out/Debug echo_worker
```

Run it

```
.\echo_server --port=5555 --engine=mojo
```

```
.\echo_worker
```