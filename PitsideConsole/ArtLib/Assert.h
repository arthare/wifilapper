void ASSERT(bool f)
{
	if(!f) __asm int 3;
}