import jxa.*;
import java.io.File;
import java.io.IOException;

class Error
{
	public static void programAlreadyInstalled (final String path)
	{
		System.err.println("already installed at " + path);
		System.exit(1);
	}
}

class Arguments
{

	public static JxaFlag<String>  task = new JxaFlag<>("task",      "task you'll be working on",    't', JxaFlag.arg.yes, "", "");
	public static JxaFlag<String>  info = new JxaFlag<>("info",      "print all/<*> your task(s)",   'i', JxaFlag.arg.may, "", "");
	public static JxaFlag<Integer> time = new JxaFlag<>("countdown", "set a time (no default mode)", 'c', JxaFlag.arg.yes);
	public static JxaFlag<Void>    help = new JxaFlag<>("help",      "print this message",           'h', JxaFlag.arg.non);
	public static JxaFlag<Void>    inst = new JxaFlag<>("install",   "install program",              'I', JxaFlag.arg.non);

	private static JxaFlag<?>[] _flags =
	{
		inst,
		task,
		time,
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

public class Main
{
	final private static String _path = System.getProperty("user.home") + "/.fourt";

	public static void main (String[] args)
	{
		Arguments.execute(args);
		if (Arguments.inst.wasSeen()) { _install(); }
	}

	private static void _install ()
	{
		/* TODO: add executable */
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
		catch (IOException e) {}
	}
}

