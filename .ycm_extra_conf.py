def FlagsForFile(fileName, **kwargs):
    return { 'flags': [ '-std=c++17',
                        '-Wall',
                        '-Werror',
                        '-Wextra',
                        '-Wpedantic' ] }


