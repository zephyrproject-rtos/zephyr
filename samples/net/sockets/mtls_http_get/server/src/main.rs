use rustls::RootCertStore;
use rustls::server::AllowAnyAuthenticatedClient;
use rustls_pemfile::{certs, rsa_private_keys};
use std::io;
use std::sync::Arc;
use tokio::io::{copy, sink, AsyncWriteExt};
use tokio::net::TcpListener;
use tokio_rustls::rustls::{self, Certificate, PrivateKey};
use tokio_rustls::TlsAcceptor;

fn load_certs() -> io::Result<Vec<Certificate>> {
    static CERT: &[u8] = include_bytes!("../server.crt");
    certs(&mut CERT.to_owned().as_slice())
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "invalid cert"))
        .map(|mut certs| certs.drain(..).map(Certificate).collect())
}

fn load_keys() -> io::Result<Vec<PrivateKey>> {
    static KEY: &[u8] = include_bytes!("../server.key");
    rsa_private_keys(&mut KEY.to_owned().as_slice())
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "invalid key"))
        .map(|mut keys| keys.drain(..).map(PrivateKey).collect())
}

fn load_root_store() -> io::Result<RootCertStore> {
    static CACERT: &[u8] = include_bytes!("../ca.crt");
    let certs = certs(&mut CACERT.to_owned().as_slice())
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "invalid ca cert"))?;
    let mut root = RootCertStore::empty();
    for crt in certs {
        root.add(&Certificate(crt))
            .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "invalid ca cert"))?;
    }
    Ok(root)
}

#[tokio::main]
async fn main() -> io::Result<()> {
    let certs = load_certs()?;
    let mut keys = load_keys()?;
    let root_store = load_root_store()?;

    let config = rustls::ServerConfig::builder()
        .with_safe_defaults()
        .with_client_cert_verifier(AllowAnyAuthenticatedClient::new(root_store))
        .with_single_cert(certs, keys.remove(0))
        .map_err(|err| io::Error::new(io::ErrorKind::InvalidInput, err))?;
    let acceptor = TlsAcceptor::from(Arc::new(config));

    const PORT: &'static str = "192.0.2.2:8443";
    let listener = TcpListener::bind(PORT).await?;
    println!("Listening on {}", PORT);

    loop {
        let (stream, peer_addr) = listener.accept().await?;
        let acceptor = acceptor.clone();

        let fut = async move {
            let mut stream = acceptor.accept(stream).await?;
            let mut output = sink();
            stream
                .write_all(
                    &b"HTTP/1.0 200 ok\r\n\
                Connection: close\r\n\
                Content-length: 13\r\n\
                \r\n\
                Hello world!\n"[..],
                )
                .await?;
            stream.shutdown().await?;
            copy(&mut stream, &mut output).await?;
            println!("Hello: {}", peer_addr);

            Ok(()) as io::Result<()>
        };

        tokio::spawn(async move {
            if let Err(err) = fut.await {
                eprintln!("{:?}", err);
            }
        });
    }
}
