import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ConnectException;
import java.net.Socket;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;

public class KlientSMTP {
	
	private static final int DLUGLINII = 1000;
	
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
		byte[] buforSerwera=new byte[KlientSMTP.DLUGLINII];
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
	
	public static void main(String[] args) {
		if (args.length!=1)
		{
			System.out.println("Błąd wywołania programu. Należy podać adres IPv4 serwera pocztowego jako pierwszy argument.");
			System.out.println("Przykład: KlientSMTP 149.156.73.222");
		}
		try {
			Socket gniazdo=new Socket(args[0], 25);
			byte[] bufor=new byte[KlientSMTP.DLUGLINII];
			int liczbaBajtow;
			String wiadomosc[]=new String[6];
			System.out.println("Witaj w programie wysyłającym jednolinijkową wiadomość.");
			System.out.println("Jak masz na imię?");
			liczbaBajtow=System.in.read(bufor);
			wiadomosc[0]=(new String(bufor,0,liczbaBajtow-1)).trim();
			wiadomosc[1]="";
			do
			{
				if(!wiadomosc[1].equals(""))
				{
					System.out.println("Adres niepoprawny.");
					wiadomosc[1]="";
				}
				System.out.println("Podaj swój adres e-mail lub zostaw puste pole.");
				liczbaBajtow=System.in.read(bufor);
				wiadomosc[1]=(new String(bufor,0,liczbaBajtow-1)).trim();
			}while((!czyPoprawnyAdres(wiadomosc[1]))&&wiadomosc[1].length()!=0);
			System.out.println("Jak nazywa się odbiorca wiadomości?");
			liczbaBajtow=System.in.read(bufor);
			wiadomosc[2]=new String(bufor,0,liczbaBajtow-1);
			wiadomosc[3]="";
			do
			{
				if(!wiadomosc[3].equals(""))
					System.out.println("Adres niepoprawny.");
				System.out.println("Podaj jego adres.");
				liczbaBajtow=System.in.read(bufor);
				wiadomosc[3]=(new String(bufor,0,liczbaBajtow-1)).trim();
			}while(!czyPoprawnyAdres(wiadomosc[3]));
			System.out.println("Jaki jest temat wiadomości?");
			liczbaBajtow=System.in.read(bufor);
			wiadomosc[4]=(new String(bufor,0,liczbaBajtow-1)).trim();			
			do
			{
				System.out.println("Podaj treść wiadomości.");
				liczbaBajtow=System.in.read(bufor);
				wiadomosc[5]=(new String(bufor,0,liczbaBajtow-1)).trim();
			}while(wiadomosc[5].length()==0);
			wyslijMail(gniazdo, wiadomosc);
			gniazdo.close();
		}
		catch (ConnectException e)
		{
			System.out.println("Połączenie odrzucone.");
		}
		catch (Exception e) 
		{ 
			e.printStackTrace(); 
		}
		System.out.println("Kończę pracę programu...");
	}
}
