import jxa.*;

class Arguments
{
	public static JxaFlag<String>  task = new JxaFlag<>("task",      "task you'll be working on",    't', JxaFlag.arg.yes, "", "");
	public static JxaFlag<String>  info = new JxaFlag<>("info",      "print all/<*> your task(s)",   'I', JxaFlag.arg.may, "", "");
	public static JxaFlag<Integer> time = new JxaFlag<>("countdown", "set a time (no default mode)", 'c', JxaFlag.arg.yes);

	private static JxaFlag<?>[] _flags =
	{
		task,
		time,
		info
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
	public static void main (String[] args)
	{
		Arguments.execute(args);
		if (Arguments.task.getArgument().isEmpty())
		{
			Arguments.usage();
		}


	}
}
