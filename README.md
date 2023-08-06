# SunShot Creator

C++ Native Node Module for SunShot (Electron) which allows for creation of stylized video files.

Make sure FFmpeg is installed with scoop and that it's bin folder is added to your PATH.
Install nlohmann-json.

```
`volta install node@16.16.0`
`npm install`
```

```
`node-gyp configure` (or with npx)
`node-gyp build`
`node test.js`
```

```
For Electron:
`node-gyp rebuild --target=21.4.4 --platform=win32 --arch=x64 --runtime=electron --abi=109 --uv=1 --libc=glibc --electron=21.4.4 --webpack=true --dist-url=https://electronjs.org/headers`
or
`node-gyp rebuild --target=21.4.4 --arch=x64 --dist-url=https://electronjs.org/headers`
```

For NPM package:
`npm publish``