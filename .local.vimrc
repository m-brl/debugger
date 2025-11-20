autocmd VimEnter,BufRead,BufNewFile *.c,*.h
    \ setlocal filetype=c | setlocal shiftwidth=2 | setlocal tabstop=2

autocmd VimEnter,BufRead,BufNewFile *.cpp,*.hpp
    \ setlocal filetype=cpp | setlocal shiftwidth=2 | setlocal tabstop=2

