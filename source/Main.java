import jxa.*;
import java.io.File;
import java.io.IOException;

final class Error
{
	public static void programAlreadyInstalled (final String path)
	{
		System.err.println("already installed at " + path);
		System.exit(1);
	}

	public static <T extends Exception> void internals (final T e)
	{
		System.err.println("internal");
		System.exit(1);
	}

	public static void emptyTaskName ()
	{
		System.err.println("empty task name");
		System.exit(1);
	}
}

final class Arguments
{
	public static JxaFlag<String>  task = new JxaFlag<>("task",      "task you'll be working on",        't', JxaFlag.arg.yes, "");
	public static JxaFlag<String>  info = new JxaFlag<>("info",      "print all/<*> your task(s)",       'i', JxaFlag.arg.may, "");
	public static JxaFlag<Integer> down = new JxaFlag<>("countdown", "countdown mode (30 mins default)", 'c', JxaFlag.arg.may, 30, 30);
	public static JxaFlag<Void>    help = new JxaFlag<>("help",      "print this message",               'h', JxaFlag.arg.non);
	public static JxaFlag<Void>    inst = new JxaFlag<>("install",   "install program",                  'I', JxaFlag.arg.non);

	private static JxaFlag<?>[] _flags =
	{
		inst,
		task,
		down,
		info,
		help,
	};

	public static void execute (final String[] args)
	{
		Jxa.parse(_flags, args);
	}

	public static void usage ()
	{
		JxaDoc.printUsage(_flags);
	}	
}

final class LogFile
{
	final private static String _path = System.getProperty("user.home") + "/.fourt";

	public static void create ()
	{
		File file = new File(_path);
		try
		{
			if (file.createNewFile())
			{
				System.out.println("program already installed :)");
				System.out.printf(" location: %s\n", file.getAbsolutePath());
			}
			else { Error.programAlreadyInstalled(_path); }
		}
		catch (IOException e) { Error.internals(e); }
	}

	public static void read ()
	{
	}

	public static void write ()
	{
	}
}

final class Information
{
	private static void _all ()
	{
		System.out.println("all tasks");
	}

	private static void _specific (final String taskname)
	{
		System.out.println("info about: " + taskname);
	}

	public static void provide ()
	{
		final String task = Arguments.info.getArgument();
		System.out.println("of: " + task);

		if (task.isEmpty()) { _all(); }
		else { _specific(task); }
	}
}

final class Task
{
	private static void _down (final String taskname)
	{
		final int min = (int) Arguments.down.getArgument();
		System.out.printf("down %d mins on %s\n", min, taskname);
	}

	private static void _up (final String taskname)
	{
		System.out.printf("working on %s\n", taskname);
	}

	public static void startTask ()
	{
		final String taskname = Arguments.task.getArgument();
		if (taskname.isEmpty())
		{
			Error.emptyTaskName();
		}

		if (Arguments.down.wasSeen()) { _down(taskname); }
		else { _up(taskname); }
	}
}

public class Main
{
	public static void main (String[] args)
	{
		Arguments.execute(args);

		     if (Arguments.inst.wasSeen()) { LogFile.create();      }
		else if (Arguments.help.wasSeen()) { Arguments.usage();     }
		else if (Arguments.info.wasSeen()) { Information.provide(); }
		else                               { Task.startTask();      }
	}
}

