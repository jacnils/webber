function get_path() {
    return window.location.pathname;
}

function set_path(path) {
    window.history.pushState({}, '', path);
}

function append_to_header(element) {
    document.getElementsByTagName('head').item(0).appendChild(element);
}

function get_cookie(name) {
    const name_eq = `${name}=`;
    const ca = document.cookie.split(';');
    for (let i = 0; i < ca.length; i++) {
        let c = ca[i];
        while (c.charAt(0) === ' ') {
            c = c.substring(1, c.length);
        }
        if (c.indexOf(name_eq) === 0) {
            return c.substring(name_eq.length, c.length);
        }
    }
    return null;
}

function include(file) {
    const script = document.createElement('script');
    script.src = file;
    script.type = 'text/javascript';
    script.defer = true;
    append_to_header(script);
}

async function get_site_settings() {
    try {
        const response = await fetch('/api/get_settings');
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        const settings = await response.json();
        return settings;
    } catch (error) {
        console.error('There was a problem with the fetch operation:', error);
    }
}

function load_page(path) {
}

function append_footer(data) {
    const footer = document.createElement('footer');
    footer.id = 'footer';
    footer.classList.add('footer');

    const p = document.createElement('p');
    p.innerText = data.footer.text;
    footer.appendChild(p);

    document.documentElement.appendChild(footer);
}

function append_header(data) {
    const scripts = () => {
        const footer = document.querySelector('footer');
        const body = document.body;
        const nav_bar = document.querySelector('.top-bar');

        if (document.body.scrollHeight <= window.innerHeight) {
            footer.classList.add('visible');
        }

        window.addEventListener('scroll', function () {
            const topBar = document.getElementById('topBar');
            const footer = document.getElementById('footer');

            if (window.scrollY > 50) {
                topBar.classList.add('hidden');
            } else {
                topBar.classList.remove('hidden');
            }

            if (window.scrollY + window.innerHeight >= document.body.scrollHeight - 50) {
                footer.classList.add('visible');
            } else {
                footer.classList.remove('visible');
            }
        });

        function set_padding() {
            const fh = footer.offsetHeight;
            body.style.paddingBottom = `${fh}px`;
        }

        function set_nav_bar_padding() {
            const nh = nav_bar.offsetHeight;
            body.style.paddingTop = `${nh}px`;
        }

        set_padding();
        set_nav_bar_padding();
        window.addEventListener('resize', set_padding);
        window.addEventListener('resize', set_nav_bar_padding);
    }

    const div = document.createElement('div');
    div.id = 'topBar';
    div.classList.add('top-bar');

    const create_div = (data, dark) => {
        const link_div = document.createElement('div');
        link_div.classList = 'header ' + (dark ? 'dark' : 'light');

        const link_a = document.createElement('a');
        link_a.href = '/';
        link_a.id = 'title';
        link_a.innerText = data.header.title;

        if (data.header.logo !== undefined) {
            const link_img = document.createElement('img');
            link_img.src = data.header.logo;
            link_img.id = 'logo';
            link_img.style.height = "100px";
            link_img.style.width = "100px";
            link_img.style.verticalAlign = "middle";
            link_img.style.padding = "10px";

            if ((dark && data.header.light_logo !== undefined && data.header.light_logo)
                || (!dark && data.header.light_logo !== undefined && !data.header.light_logo)) {
                link_img.style.filter = "invert(1)";
            }

            link_a.innerHTML = '';
            link_a.appendChild(link_img);
            link_a.appendChild(document.createTextNode(data.header.title));
        }

        link_div.appendChild(link_a);
        div.appendChild(link_div);
    }

    create_div(data, false);
    create_div(data, true);

    if (data.header.links !== undefined) {
        const nav = document.createElement("div");
        nav.classList.add('nav-links');
        nav.id = 'navLinks';

        let logged_in = false;
        let admin = false;

        if (get_cookie('username') !== null && get_cookie('user_type') !== null) {
            logged_in = true;
        }
        if (get_cookie('user_type') === 1) {
            admin = true;
        }

        for (const link of data.header.links) {
            if ((link.auth !== undefined && link.auth) && !logged_in) {
                continue;
            } else if ((link.admin !== undefined && link.admin) && !admin) {
                continue;
            } else if ((link.auth !== undefined && !link.auth) && logged_in) {
                continue;
            } else if ((link.admin !== undefined && !link.admin) && admin) {
                continue;
            }

            const a = document.createElement('a');
            a.innerText = link.name;
            a.onclick = () => {
                set_path(link.path);
                load_page(link.path);
            }
            if (link.id !== undefined) {
                a.id = link.id;
            }
            if (link.class !== undefined) {
                a.classList.add(link.class);
            }
            if (link.href !== undefined) {
                a.href = link.href;
            }
            nav.appendChild(a);
        }

        div.appendChild(nav);
    }

    document.body.appendChild(div);

    scripts();
}

function set_defaults(data) {
    document.title = data.default_site.title;

    const meta_description = document.createElement('meta');
    meta_description.name = 'description';
    meta_description.content = data.default_site.description;
    document.head.appendChild(meta_description);

    const meta_keywords = document.createElement('meta');
    meta_keywords.name = 'keywords';
    meta_keywords.content = data.default_site.keywords;
    document.head.appendChild(meta_keywords);

    const meta_author = document.createElement('meta');
    meta_author.name = 'author';
    meta_author.content = data.default_site.author;
    document.head.appendChild(meta_author);

    const meta_viewport = document.createElement('meta');
    meta_viewport.name = 'viewport';
    meta_viewport.content = data.default_site.viewport;
    document.head.appendChild(meta_viewport);

    const link_favicon = document.createElement('link');
    link_favicon.rel = 'icon';
    link_favicon.href = data.default_site.favicon;
    document.head.appendChild(link_favicon);

    const css = document.createElement('link');
    css.rel = 'stylesheet';
    css.href = '/css/main.css';
    document.head.appendChild(css);
}

function main(data) {
    set_defaults(data);
    append_footer(data);
    append_header(data);

    const div = document.createElement('div');
    div.id = 'content';
    div.innerHTML = 'Hello world!';
}

document.addEventListener('DOMContentLoaded', function() {
    include("https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/highlight.min.js");
    include("https://kit.fontawesome.com/aa55cd1c33.js");

    get_site_settings().then((data) => {
        main(data);
    });
})