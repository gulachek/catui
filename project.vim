set path=,,src/**,include/**,test/**
nnoremap <F5> :!node make.mjs serve<CR>

augroup msgstream
	autocmd!
	autocmd BufNewFile *.c,*.h,*.cpp,*.hpp :0r <sfile>:h/vim/template/skeleton.c
augroup END
