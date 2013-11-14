
{
  'targets': [
    {
      'target_name': 'TileMill',
      'product_name': 'TileMIll',
      'type': 'executable',
      'product_prefix': '',
      'product_extension':'exe',
      'sources': [
        'tilemill.cc',
      ],
      'defines': [
        'PLATFORM="<(OS)"',
      ],
      'conditions': [
        [ 'OS=="win"', {
          'defines': [
            'PLATFORM="win32"',
            '_LARGEFILE_SOURCE',
            '_FILE_OFFSET_BITS=64',
            '_WINDOWS',
            'BUILDING_NODE_EXTENSION'
          ],
          'libraries': [
            '-luser32.lib', # MessageBox
            '-lshell32.lib', # GetSpecialFolderPath
          ],
          'include_dirs': [
          ],
          'msvs_settings': {
            'VCLinkerTool': {
			  'target_conditions': [
			     ['_type=="executable"', {
                     'SubSystem': 2,
				 }],
			  ],
              'AdditionalLibraryDirectories': [
              ],
            },
			'VCResourceCompilerTool' : {
			      'PreprocessorDefinitions': ["_WINDOWS"]
			},
			},
        },
      ], # windows
      ] # condition
    }, # targets
  ],
}