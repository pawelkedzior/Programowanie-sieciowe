import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ConnectException;
import java.net.Socket;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class DNS {
	
	private static final int DLUGLINII = 1000;
	
	public static native String[] adresyIP(String adresSerwera);
	
	static
	{
		System.loadLibrary("otrzymajAdresy");
	}
	
	private static boolean czyPoprawnyAdres(String adres)
	{
		if(!adres.contains("@"))
			return false;
		String podzial[]=adres.split("@");
		if(!podzial[1].contains("."))
			return false;
		return true;
	}
	
	private static String obecnaData()
	{
		String data="";
		DateFormat formatDaty=new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss Z");
        data=data+formatDaty.format(Calendar.getInstance().getTime());
		return data;
	}
	
	public static String stworzStruktureWiadomosci(String nadawca, String mailNadawcy, String adresat, 
			String mailAdresata, String temat)
	{
		String strukturaWiadomosci="From: ["+nadawca+"] <"+mailNadawcy+">\r\n";
		strukturaWiadomosci=strukturaWiadomosci+"To: ["+adresat+"] <"+mailAdresata+">\r\n";
		strukturaWiadomosci=strukturaWiadomosci+"Subject: "+temat+"\r\n";
		strukturaWiadomosci=strukturaWiadomosci+"Date: "+obecnaData()+"\r\n";
		strukturaWiadomosci=strukturaWiadomosci+"Content-Type: text/plain; charset=UTF-8\r\n\r\n";
		return strukturaWiadomosci;
	}
	
	public static void wyslijMail(Socket gniazdo, String[] wiadomosc) throws IOException
	{
		byte[] buforSerwera=new byte[DNS.DLUGLINII];
		InputStream wejscie=gniazdo.getInputStream();
		OutputStream wyjscie=gniazdo.getOutputStream();
		int liczbaBajtow;
		
		wejscie.read(buforSerwera);
		if((new String(buforSerwera).substring(0, 3)).equals("220"))
		{
			System.out.println("Połączyłem się z serwerem.");
		}
		else
		{
			System.out.println("Błąd przy połączeniu z serwerem.");
			wyjscie.write("QUIT\r\n".getBytes());
			return;
		}
		String instrukcja="HELO "+gniazdo.getInetAddress().toString().substring(1)+"\r\n";
		wyjscie.write(instrukcja.getBytes());
		wejscie.read(buforSerwera);
		if((new String(buforSerwera).substring(0, 3)).equals("250"))
		{
			instrukcja="MAIL FROM:<"+wiadomosc[1]+">\r\n";
			wyjscie.write(instrukcja.getBytes());
			wejscie.read(buforSerwera);
			if(!(new String(buforSerwera).substring(0, 3)).equals("250"))
			{
				System.out.println("Serwer nie przyjmuje wiadomości z tego adresu.");
				wyjscie.write("QUIT\r\n".getBytes());
				return;				
			}
			instrukcja="RCPT TO:<"+wiadomosc[3]+">\r\n";
			wyjscie.write(instrukcja.getBytes());
			liczbaBajtow=wejscie.read(buforSerwera);
			String adr=new String(buforSerwera,0,liczbaBajtow-1);
			if(!adr.substring(0,3).equals("250")/*!(new String(buforSerwera).substring(0, 3)).equals("250")*/)
			{
				System.out.println("Dany adresat nie znajduje się w bazie serwera.");
				System.out.println(adr);
				wyjscie.write("QUIT\r\n".getBytes());
				return;				
			}
			instrukcja="DATA\r\n";
			wyjscie.write(instrukcja.getBytes());
			wejscie.read(buforSerwera);
			if(!(new String(buforSerwera).substring(0, 3)).equals("354"))
			{
				System.out.println("Serwer odmówił wysyłania wiadomości.");
				wyjscie.write("QUIT\r\n".getBytes());
				return;				
			}
			instrukcja=stworzStruktureWiadomosci(wiadomosc[0],wiadomosc[1],wiadomosc[2],wiadomosc[3],wiadomosc[4]);
			instrukcja=instrukcja+wiadomosc[5]+"\r\n\r\n.\r\n";
			wyjscie.write(instrukcja.getBytes());
			liczbaBajtow=wejscie.read(buforSerwera);
			String komunikat=new String(buforSerwera,0,liczbaBajtow-1);
			if(komunikat.substring(0,3).equals("250"))
			{
				System.out.println("Wiadomość wysłana.");
			}
			else
			{
				System.out.println("Błąd wysyłania wiadomości: "+komunikat.substring(4));
			}
		}
		else
		{
			System.out.println("Błąd przywitania.");
		}
		instrukcja="QUIT\r\n";
		wyjscie.write(instrukcja.getBytes());
		wejscie.read(buforSerwera);
	}
	
	public static boolean sprawdzMail(String odbiorca) throws Exception
	{
		System.out.println("Sprawdzam adres: "+odbiorca);
		String czesciAdresu[]=odbiorca.split("@");
		String adresyIP[];
		try
		{
			adresyIP=adresyIP(czesciAdresu[1]);
		}
		catch (NullPointerException e)
		{
			if(!e.getMessage().equals("malloc: out of memory\n"))
				throw new Exception("Nie znaleziono żadnego adresu IP dla tej domeny");
			else
				throw new NullPointerException("Błąd przydzielania pamięci.");
		}
		System.out.println("Znaleziono adresy IP w liczbie: "+adresyIP.length);
				
		for (String adres: adresyIP) 
		{
			Socket gniazdo=new Socket(adres, 25);
			
			byte[] buforSerwera=new byte[DNS.DLUGLINII];
			InputStream wejscie=gniazdo.getInputStream();
			OutputStream wyjscie=gniazdo.getOutputStream();
			int liczbaBajtow;
			
			wejscie.read(buforSerwera);
			if(!(new String(buforSerwera).substring(0, 3)).equals("220"))
			{
				wyjscie.write("QUIT\r\n".getBytes());
				gniazdo.close();
				throw new IOException("Błąd przy połączeniu z serwerem.");
			}
			String instrukcja="HELO "+gniazdo.getInetAddress().toString().substring(1)+"\r\n";
			wyjscie.write(instrukcja.getBytes());
			wejscie.read(buforSerwera);
			if((new String(buforSerwera).substring(0, 3)).equals("250"))
			{
				instrukcja="MAIL FROM:<>\r\n";
				wyjscie.write(instrukcja.getBytes());
				wejscie.read(buforSerwera);
				if(!(new String(buforSerwera).substring(0, 3)).equals("250"))
				{
					wyjscie.write("QUIT\r\n".getBytes());
					gniazdo.close();
					throw new Exception("Serwer nie chce pozwolić na sprawdzenie.");
				}
				instrukcja="RCPT TO:<"+odbiorca+">\r\n";
				wyjscie.write(instrukcja.getBytes());
				liczbaBajtow=wejscie.read(buforSerwera);
				String adr=new String(buforSerwera,0,liczbaBajtow-1);
				
				if(adr.substring(0,3).equals("250"))
				{
					wyjscie.write("QUIT\r\n".getBytes());
					gniazdo.close();
					return true;				
				}
				else
				{
					wyjscie.write("QUIT\r\n".getBytes());
					gniazdo.close();				
				}
			}
			else
			{
				instrukcja="QUIT\r\n";
				wyjscie.write(instrukcja.getBytes());
				wejscie.read(buforSerwera);
				gniazdo.close();
				System.out.println("Błąd przywitania z serwerem o adresie "+adres);
			}
		}
		return false;
	}
	
	public static void main(String[] args) 
	{
		System.out.println("Witaj w programie sprawdzającym podane przez Ciebie konta pocztowe.");
		System.out.println();
		System.out.println();
		System.out.println("Rozpoczynam pracę.");
		System.out.println();
		
		for (int i=0;i<args.length;i++)
		{
			try {
				if (!czyPoprawnyAdres(args[i]))
					throw new Exception("Adres: "+args[i]+" nie jest poprawnym adresem mail.");
				
				if (sprawdzMail(args[i]))
					System.out.println("Adres przyjęty.");
				else
					System.out.println("Adres odrzucony.");
				System.out.println();
			}
			catch (ConnectException e)
			{
				System.out.println("Połączenie odrzucone.");
			}
			catch (Exception e) 
			{ 
				System.out.println("Błąd: "+e.getMessage());
			}
		}
		System.out.println("Kończę pracę programu...");
	}
}
