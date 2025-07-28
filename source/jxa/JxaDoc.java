package jxa;

public class JxaDoc
{
	private static int _longestLongName = 0;
	private static int _longestFlagDesc = 0;
	
	/* If you as a programmer use this function i highly suggest you
	 * to pipe this output to column GNU/Linux command for better reading
	 * ... | column -t -s ':'
	 */
	public static void debugInfo (final JxaFlag<?>[] flags)
	{
		System.out.println("Flags");
		for (int i = 0; i < flags.length; i++)
		{
			JxaFlag<?> f = flags[i];
			System.out.println("  " + f.getLongname() + ":" + f.getShortname() + ":" + f.wasSeen() + ":" + f.getArgument());
		}
		System.out.println("\nPositional Arguments");
		for (int i  = 0; i < Jxa._posArguments.size(); i++)
		{	
			System.out.println("  " + Jxa._posArguments.get(i));
		}
	}
	
	public static void printUsage (final JxaFlag<?>[] flags)
	{
		getLongss(flags);
		_longestFlagDesc += 2;
		_longestLongName += 2;
		
		System.out.println("    \nUsage: " + JxaInfo.projectName + " - " + JxaInfo.projectDesc + "\n");
		System.out.println("    flags:");
		for (int i = 0; i < flags.length; i++)
		{
			final JxaFlag<?> f = flags[i];
			final String fmt = String.format(
				"      -%c or --%-" + _longestLongName + "s" + "%-" + _longestFlagDesc + "s - %s",
				f.getShortname(),
				f.getLongname(),
				f.getDescription(),
				_getEnumAsStr(f.getArgTaker())
			);
			System.out.println(fmt);
		}
		System.out.println("");
	}
	
	private static String _getEnumAsStr (final JxaFlag.arg a)
	{
		switch (a)
		{
			case yes: return "needs argument";
			case non: return "does not require argument";
			default:  return "argument is optional";
		}
	}
	
	private static void getLongss (final JxaFlag<?>[] flags)
	{	
		for (int i = 0; i < flags.length; i++)
		{
			final int nameLen = flags[i].getLongname().length();
			final int descLen = flags[i].getDescription().length();
			
			_longestLongName = (_longestLongName > nameLen) ? _longestLongName : nameLen;
			_longestFlagDesc = (_longestFlagDesc > descLen) ? _longestFlagDesc : descLen;
		}
	}	
}