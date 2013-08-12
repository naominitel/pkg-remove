extern int pkg_remove(char const *fname);

int main()
{
    char const* fname = "com.apple.pkg.BSD";
    return pkg_remove(fname);
}
