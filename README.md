# SunShot Creator

C++ Native Node Module for SunShot (Electron) which allows for creation of video files which follow the users mouse.

```
`volta install node@16.16.0`
Install ffmpeg-shared with scoop
Install nlohmann-json:x86-windows with vcpkg
Update binding.gyp with your file paths
`npm install`
Will probably need Windows 10 SDK
Make sure C:/Users/alext/scoop/apps/ffmpeg-shared/7.0 is in your PATH or you'll get the dll load error
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