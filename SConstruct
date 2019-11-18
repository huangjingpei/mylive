

Import('*')

_myName = 'librtspclient'




env = Environment()  
_env = env.Clone()
_env.Append(CPPPATH = ['#.',
		'#liveMedia/include',
		'#groupsock/include',
		'#UsageEnvironment/include',
		'#BasicUsageEnvironment/include',			
		])

_env.Append(LIBPATH = [
		'#liveMedia',
		'#groupsock',
		'#UsageEnvironment',
		'#BasicUsageEnvironment'	
		])
		

_env.Append(LIBS = [
	'liveMedia', 
	'groupsock', 
	'UsageEnvironment',
	'BasicUsageEnvironment',
	#'fadd'
	])


_env.Append(CPPFLAGS = ['-D_LINUX', '-DLINUX'])
_env.Append(LDFLAGS = ['-shared -fPIC -shared'])
totalSources = '''
		#RtspClient/AACDecoder.cpp \
		#RtspClient/H264Decoder.cpp \
		RtspClient/lstLib.cpp \
		RtspClient/SpecificData.cpp \
		RtspClient/BitStream.cpp \
		RtspClient/LiveRtspClient.cpp \
		
	'''
objs = totalSources.split()
target = _env.SharedLibrary(_myName + '.so', objs, _LIBFLAGS=' -Wl,-Bsymbolic')
objs.append(target)
#target = _env.Command(_myName + '.so', _myName + '.so-dev', "$STRIP -g $SOURCE -o $TARGET")
#objs.append(target)

all = target
Return('all')

#_env = env.Clone()
#_env.Append(CPPPATH = ['#./'
#		'#librtsp/inc'])
#
#_env.Append(CPPFLAGS = ['-D_LINUX', '-DLINUX'])
#_env.Append(LIBPATH = ['#release/lib'])
#_env.Append(LIBS = ['rtsp', 'pthread', 'rt'])
#execname = 'test_rtsp'
#execObj = Glob('test/*.cpp')
#target += _env.Program(execname, execObj);
#all = target
#Return('all')
