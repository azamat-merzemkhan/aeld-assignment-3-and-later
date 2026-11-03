require 'socket'
require 'tmpdir'

# This receiver is used only in the isolated GitHub Actions test container.
abort 'A syslog socket already exists; refusing to replace it' if File.exist?('/dev/log')
receiver = Socket.new(Socket::AF_UNIX, Socket::SOCK_DGRAM, 0)
receiver.bind(Socket.sockaddr_un('/dev/log'))

begin
  Dir.mktmpdir('writer-syslog-') do |directory|
    target = File.join(directory, 'test.txt')
    abort 'writer failed' unless system('./finder-app/writer', target, 'syslog-check')
    abort 'No debug log received' unless IO.select([receiver], nil, nil, 5)
    debug_message = receiver.recv(8192)
    abort "Incorrect debug log: #{debug_message}" unless
      debug_message.start_with?('<15>') &&
      debug_message.include?("Writing syslog-check to #{target}")

    abort 'writer unexpectedly succeeded' if system('./finder-app/writer')
    abort 'Incorrect error return code' unless $?.exitstatus == 1
    abort 'No error log received' unless IO.select([receiver], nil, nil, 5)
    error_message = receiver.recv(8192)
    abort "Incorrect error log: #{error_message}" unless error_message.start_with?('<11>')
    puts "LOG_USER/LOG_DEBUG: #{debug_message}"
    puts "LOG_USER/LOG_ERR: #{error_message}"
  end
ensure
  receiver.close
  File.unlink('/dev/log')
end
