{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "lock.c", "os-lock.cc" ],
      "defines": [ "_get_osfhandle=uv_get_osfhandle" ]
    }
  ]
}
