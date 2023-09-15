target remote 127.0.0.1:1234
b start
layout split
b entry
b __panic
b init_paging
b test_alloc_pages
b alloc_pages
b free_pages
