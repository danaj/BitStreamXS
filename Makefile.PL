use ExtUtils::MakeMaker;
WriteMakefile(
    NAME         => 'Data::BitStream::BitList',
    ABSTRACT     => 'A class implementing a stream of bits and coding methods',
    VERSION_FROM => 'lib/Data/BitStream/BitList.pm',

    ($ExtUtils::MakeMaker::VERSION >= 6.3002
      ? ('LICENSE'=> 'perl') : ()),

    ($] >= 5.005 ?     ## Add these new keywords supported since 5.005
       (AUTHOR         => 'Dana A Jacobsen <dana@acm.org>') : ()),

    LIBS         => [''],   # e.g., '-lm' 
    DEFINE       => '',     # e.g., '-DHAVE_SOMETHING' 
    INC          => '',     # e.g., '-I/usr/include/other' 
    OBJECT       => 'bitlist.o BitList.o',

    PREREQ_PM => {
                    'Test::More'       => '0.47',
                    'Exporter'         => '5.562',
                 },
);
