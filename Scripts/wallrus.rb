#!/usr/bin/env ruby
# frozen_string_literal: true

################################################################################
# begin script configuration options
################################################################################

# which workspaces should the script manage
$image_dirs = {
  1 =>
    ['/storage/owncloud/Images/astronomy'],
  2 =>
    ['/storage/owncloud/Images/wallpapers',
     '/storage/owncloud/Images/a_vintage_journey',
     '/storage/owncloud/Images/ben-regimbal',
     '/storage/owncloud/Images/chahin-lowpoly',
     '/storage/owncloud/Images/mc_escher',
     '/storage/owncloud/Images/jia-yonghui',
     '/storage/owncloud/Images/townscaper'],
  3 =>
    ['/storage/owncloud/Images/Futurama/wallpapers']
}

# time in seconds between image rotation
SLEEP_TIME = 3600

# specify the path to bgswitch or comment out if it's already in $PATH
BGSWITCH = '/storage/projects/bgswitch/build/bgswitch'

# Experimental! Do not enable!
# use BFS queries to find backgrounds
# QUERY_MODE = true

# TODO: comment out to disable logging
LOG_FILE = '/var/log/wallrus.log'

################################################################################
# set up signal handling to change wallpapers on demand with:
#   wallrus.rb -n
# or
#   kill -USR1 `cat /tmp/wallrus.rb.pid`
################################################################################

PID_FILE = "/tmp/#{File.basename($PROGRAM_NAME)}.pid"

class BackgroundChange < StandardError
end

# handle SIGUSR1 in order to change backgrounds on demand
Signal.trap('USR1') { raise BackgroundChange }

################################################################################
# main code for the script
################################################################################

require 'logger'

# needed for shellescape
require 'shellwords'

@log = Logger.new(LOG_FILE)

# override formatter to log to STDOUT and LOG_FILE
@log.formatter = proc do |severity, datetime, _progname, message|
  puts "#{severity}: [#{datetime} PID:#{Process.pid}] #{message}\n"
  "#{severity}: [#{datetime} PID:#{Process.pid}] #{message}\n"
end

def rescan(workspace, bg_slice)
  print "Gathering files for workspace #{workspace} "
  $image_dirs[workspace].each do |dir|
    print '.'
    bg_slice.concat(
      if defined?(QUERY_MODE) && QUERY_MODE
        `query -f -v "#{dir}" '((name=*)&&(BEOS:TYPE=image/*))' 2>/dev/null`.split("\n").select { |d| /#{dir}#{'/' unless dir.end_with?('/')}/ === d }
      else
        # TODO: filter for image files
        Dir["#{dir}/*"].select { |f| File.file?(f) }
      end
    )
  end
  puts " #{bg_slice.length} found"
  @log.info("#{bg_slice.length} backgrounds found for workspace #{workspace}")
end

def rotate(workspace, bg_slice)
  rescan(workspace, bg_slice) if bg_slice.empty?

  if bg_slice.empty?
    # if it's still empty then remove it
    @log.info("No backgrounds for workspace #{workspace}. Removing from list.")
    $image_dirs.delete(workspace)
    return
  end

  # use sample to get a random entry and then delete it from the list
  the_wall = bg_slice.sample
  bg_slice.delete(the_wall)

  @log.info("Workspace #{workspace} [#{bg_slice.length} left]: #{the_wall}")

  # execute our bgswitch command
  `#{defined?(BGSWITCH) ? BGSWITCH : 'bgswitch'} -w #{workspace} set #{defined?(QUERY_MODE) && QUERY_MODE ? the_wall : the_wall.shellescape}`
  # TODO: check error code from bgswitch
end

def start_rotation
  # write our PID to a file
  File.write(PID_FILE, $$)
  at_exit { File.delete(PID_FILE) if File.exist?(PID_FILE) }

  backgrounds = []

  loop do
    $image_dirs.each do |space, _dirs|
      backgrounds[space] = [] if backgrounds[space].nil?
      rotate(space, backgrounds[space])
    end

    sleep SLEEP_TIME

  # interrupt sleep and change backgrounds
  rescue BackgroundChange

  # handle Ctrl-C interrupt nicely
  rescue Interrupt
    @log.info('Shutting down...')
    @log.close
    break
  end
end

require 'optparse'

parser = OptionParser.new do |opts|
  opts.banner = "Usage: #{$PROGRAM_NAME} [-n or -s]"

  opts.on('-n', '--next', 'Switch to next background') do
    if File.exist?(PID_FILE)
      print 'Switching ....'
      Process.kill('USR1', `cat #{PID_FILE}`.to_i)
      puts ' done'
    else
      puts "#{$PROGRAM_NAME} doesn't appear to be running. #{PID_FILE} not found"
    end
  end

  opts.on('-s', '--start', 'Start rotating backgrounds') do
    if File.exist?(PID_FILE)
      @log.error("#{PID_FILE} exists. Delete the file if #{$PROGRAM_NAME} is not running")
      exit(2)
    end
    start_rotation
  end

  opts.on('-h', '--help', 'Show this help message') do
    puts opts
  end
end

# print the usage if there were no args
ARGV << '-h' if ARGV.empty?

if ARGV.length > 1 || (ARGV.length == 1 && ARGV[0].length > 2 && /--/ !~ ARGV[0])
  puts 'Error: Too many arguments'
  puts parser
  exit(1)
end

begin
  parser.parse!
rescue OptionParser::InvalidOption, OptionParser::MissingArgument => e
  puts e
  puts parser
end
