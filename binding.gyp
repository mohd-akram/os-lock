{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "lock.c", "addon.c", "addon-node.c" ],
      "defines": [ "_get_osfhandle=uv_get_osfhandle" ]
    }
  ]
}
