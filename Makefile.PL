use ExtUtils::MakeMaker;
WriteMakefile(
    'NAME'	=> 'Data::BitStream::BitList',
    'VERSION_FROM' => 'lib/Data/BitStream/BitList.pm',
    'LIBS'	=> [''],   # e.g., '-lm' 
    'DEFINE'	=> '',     # e.g., '-DHAVE_SOMETHING' 
    'INC'	=> '',     # e.g., '-I/usr/include/other' 
    'OBJECT'    => 'bitlist.o BitList.o'
);
