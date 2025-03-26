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

function clear_cookies() {
    const cookies = document.cookie.split(';');
    for (let i = 0; i < cookies.length; i++) {
        const cookie = cookies[i];
        const eq = cookie.indexOf('=');
        const name = eq > -1 ? cookie.substr(0, eq).trim() : cookie.trim();
        document.cookie = `${name}=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/; domain=${window.location.hostname}`;
    }
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
            throw new Error('network response was not ok');
        }
        const settings = await response.json();
        return settings;
    } catch (error) {
        console.error('there was a problem with the fetch operation:', error);
    }
}

class WindowProperties {
    constructor({
                    close_button = true,
                    moveable = false,
                    close_on_click_outside = false,
                    close_on_escape = true,
                    remove_existing = true,
                    function_on_close = null
                } = {}) {
        this.close_button = close_button;
        this.moveable = moveable;
        this.close_on_click_outside = close_on_click_outside;
        this.close_on_escape = close_on_escape;
        this.remove_existing = remove_existing;
        this.function_on_close = function_on_close;
    }
}

function hide_all_windows() {
    const windows = document.getElementsByClassName('floating_window');
    for (let i = 0; i < windows.length; i++) {
        windows[i].style.display = 'none';
        while (windows[i].firstChild) {
            windows[i].removeChild(windows[i].firstChild);
        }
    }
}

function create_window(id, prop = new WindowProperties()) {
    const hide_all_windows = () => {
        const windows = document.getElementsByClassName('floating_window');
        for (let i = 0; i < windows.length; i++) {
            windows[i].style.display = 'none';
            while (windows[i].firstChild) {
                windows[i].removeChild(windows[i].firstChild);
            }
        }
    }
    if (prop.remove_existing) {
        hide_all_windows();
    }

    const existing = document.getElementById(id);
    if (existing) {
        existing.style.display = 'none';
        while (existing.firstChild) {
            existing.removeChild(existing.firstChild);
        }
    }
    const content = document.createElement('div');
    content.className = 'floating_window';
    content.id = id;
    if (prop.close_on_click_outside) {
        content.onclick = (event) => {
            if (event.target === content) {
                if (prop.function_on_close) {
                    prop.function_on_close();
                    return;
                }
                hide_all_windows();
            }
        }
    }
    if (prop.close_on_escape) {
        document.onkeydown = (event) => {
            if (event.key === 'Escape') {
                if (prop.function_on_close) {
                    prop.function_on_close();
                    return;
                }
                hide_all_windows();
            }
        }
    }

    content.oncontextmenu = (event) => {
        event.preventDefault();
    }

    let xpos = 0;
    let ypos = 0;
    let offsetX = 0;
    let offsetY = 0;

    if (prop.moveable) {
        content.onmousedown = (event) => {
            xpos = event.clientX;
            ypos = event.clientY;
            offsetX = xpos - content.offsetLeft;
            offsetY = ypos - content.offsetTop;

            document.onmousemove = (event) => {
                content.style.left = (event.clientX - offsetX) + 'px';
                content.style.top = (event.clientY - offsetY) + 'px';
            }
        }

        document.onmouseup = () => {
            document.onmousemove = null;
        }
    }

    if (prop.close_button) {
        const close = document.createElement('a');
        close.innerHTML = '✕';
        close.id = 'window-close';
        close.style.position = 'fixed';
        close.style.padding = '10px';
        close.style.top = '0';
        close.style.right = '0';
        close.style.textDecoration = 'none';
        close.onclick = () => {
            if (prop.function_on_close) {
                prop.function_on_close();
                return;
            }
            hide_all_windows();
        }

        content.appendChild(close);
    }

    const content_div = document.getElementById('content');
    if (content_div) {
        content_div.appendChild(content);
    }

    return content;
}

async function is_username(username) {
    const api = "/api/user_exists";
    const data = { username: username };

    try {
        const response = await fetch(api, {
            headers: {
                'Content-Type': 'application/json'
            },
            method: 'POST',
            body: JSON.stringify(data),
        });
        const result = await response.json();
        return result.status !== undefined ? result.status : true;
    } catch (error) {
        console.error('Error:', error);
        return true;
    }
}

async function login(data, error = "") {
    const try_login = (data, username, password) => {
        let send = {};
        if (username.includes('@')) {
            send.email = username;
        } else {
            send.username = username;
        }
        send.password = password;

        fetch("/api/try_login", {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(send),
        })
            .then(response => response.json())
            .then(json => {
                if (json.error_str) {
                    login(data, json.error_str);
                    return;
                } else if (json.key) {
                    window.location.href = '/';
                    return;
                }

                throw new Error('Invalid response from server: ' + JSON.stringify(json));
            })
            .catch((error) => {
                console.error('Error:', error);
            });
    }

    const password_prompt = () => {
        const username = document.getElementById('login_username').value;

        const window = create_window('login-window');
        const title = document.createElement('h1');
        title.id = 'login_title';
        title.classList = 'login-title';
        title.innerText = 'Enter your password!';

        const paragraph = document.createElement('p');
        paragraph.id = 'login_paragraph';
        paragraph.classList = 'login-paragraph';
        paragraph.innerText = 'Please enter your password to continue.';

        const button = document.createElement('button');
        button.id = 'login_button';
        button.innerText = 'Login';
        button.enabled = false;
        button.onclick = (event) => {
            try_login(data, username, password.value);
        }

        const password = document.createElement('input');
        password.id = 'login_password';
        password.classList = 'login-password';
        password.type = 'password';
        password.placeholder = 'Password';
        password.oninput = () => {
            button.enabled = password.value !== "";
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(password);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const username_prompt = async () => {
        const window = create_window('login-window');
        const title = document.createElement('h1');
        title.id = 'login_title';
        title.classList = 'login-title';
        title.innerText = 'Welcome back!';

        const paragraph = document.createElement('p');
        paragraph.id = 'login_paragraph';
        paragraph.classList = 'login-paragraph';
        paragraph.innerHTML = 'Welcome back to ' + data.general.site + '. Please login to continue.<br>';
        const link = document.createElement('a');
        link.onclick = () => {
            register(data);
        }
        link.id = 'login_register_link';
        link.classList = 'login-register-link';
        link.innerText = "Don't have an account? Register here!";
        paragraph.appendChild(link);

        const username = document.createElement('input');
        username.id = 'login_username';
        username.type = 'text';
        username.placeholder = 'Username or email address';
        username.oninput = () => {
            const errors = document.getElementsByClassName('error');
            for (let i = 0; i < errors.length; i++) {
                const error = errors[i];
                window.removeChild(error);
            }
        }

        const button = document.createElement('button');
        button.id = 'login_button';
        button.innerText = 'Continue';
        button.enabled = false;
        button.onclick = () => {
            password_prompt();
        }

        username.oninput = async () => {
            if (username.value === "") {
                button.enabled = false;
            } else {
                button.enabled = true;
            }
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(username);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);

        if (error !== "") {
            const error_message = document.createElement('p');
            error_message.id = 'login-error';
            error_message.classList = 'error login-error';
            error_message.innerText = error;
            window.appendChild(error_message);
        }
    }

    username_prompt();
}

function logout(data) {
    const win = create_window('logout-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerText = 'Log out';
    title.id = 'logout-title';
    title.classList = 'logout_title';

    paragraph.innerText = 'Are you sure you want to log out?';
    paragraph.id = 'logout-paragraph';
    paragraph.classList = 'logout_paragraph';

    const button = document.createElement('button');
    button.innerHTML = 'Yes, log me out!';
    button.onclick = () => {
        clear_cookies();
        window.location = "/";
    }

    win.appendChild(title);
    win.appendChild(paragraph)
    win.appendChild(button);
}

function register(data, error = "") {
    let stat = {};

    const try_register = async () => {
        const api = "/api/register";
        const send = {
            username: stat.username,
            email: stat.email,
            password: stat.password
        }


        fetch("/api/try_register", {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(send),
        })
            .then(response => {
                if (response.status === 204) { // success without email verification needed
                    hide_all_windows();
                    return;
                }

                return response.json();
            })
            .then(json => {
                if (json && json.error_str) {
                    register(data, json.error_str);
                    return;
                }

                if (json) {
                    throw new Error('Invalid response from server: ' + JSON.stringify(json));
                }
            })
            .catch((error) => {
                console.error('Error:', error);
            });
    }

    const confirm_prompt = async () => {
        stat.password = document.getElementById('register_password').value;

        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Confirm your password';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Please re-enter the password you just entered. This is to ensure that you did not make any mistakes, and that you can remember your password.';

        const confirm = document.createElement('input');
        confirm.id = 'register_confirm';
        confirm.classList = 'register-confirm';
        confirm.type = 'password';
        confirm.placeholder = 'Confirm your password';

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Register';

        button.onclick = async () => {
            if (confirm.value !== stat.password) {
                register(data, 'Passwords do not match.');
                return;
            }

            try_register();
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(confirm);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const password_prompt = async () => {
        stat.email = document.getElementById('register_email').value;

        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Password';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Please enter a password for this account. A good password is at least 8 characters long and contains a mix of letters, numbers, and symbols. Please do not use a password you use elsewhere or have used in the past.';

        const password = document.createElement('input');
        password.id = 'register_password';
        password.classList = 'register-password';
        password.type = 'password';
        password.placeholder = 'Password';

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Continue';

        button.onclick = async () => {
            if (password.value === "") {
                register(data, 'Please enter a password.');
                return;
            } else if (password.value.length < 8) {
                register(data, 'Password must be at least 8 characters long.');
                return;
            }

            confirm_prompt();
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(password);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const email_prompt = async () => {
        stat.username = document.getElementById('register_username').value;

        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Email address';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Please enter your email address. It will be used in the event that we need to urgently contact you. No spam, we promise!';

        const email = document.createElement('input');
        email.id = 'register_email';
        email.classList = 'register-email';
        email.type = 'email';
        email.placeholder = 'Email address';

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Continue';
        button.onclick = async () => {
            if (email.value === "") {
                register(data, 'Please enter your email address.');
                return;
            } else if (email.value.includes('@') == false) {
                register(data, 'That does not look like an email address.');
                return;
            } else if (await is_username(email.value)) {
                register(data, 'Email address already exists.');
                return;
            }

            password_prompt();
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(email);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);
    }

    const username_prompt = async () => {
        const window = create_window('register-window');
        const title = document.createElement('h1');
        title.id = 'register_title';
        title.classList = 'register-title';
        title.innerText = 'Welcome!';

        const paragraph = document.createElement('p');
        paragraph.id = 'register_paragraph';
        paragraph.classList = 'register-paragraph';
        paragraph.innerHTML = 'Welcome to ' + data.general.site + '.<br>';
        const link = document.createElement('a');
        link.onclick = () => {
            register(data);
        }
        link.id = 'register_login_link';
        link.classList = 'register-login-link';
        link.innerText = "Already have an account? Log in here!";
        paragraph.appendChild(link);

        const username = document.createElement('input');
        username.id = 'register_username';
        username.type = 'text';
        username.placeholder = 'Username';
        username.oninput = () => {
            const errors = document.getElementsByClassName('error');
            for (let i = 0; i < errors.length; i++) {
                const error = errors[i];
                window.removeChild(error);
            }
        }

        const button = document.createElement('button');
        button.id = 'register_button';
        button.innerText = 'Continue';
        button.enabled = false;
        button.onclick = async () => {
            if (await is_username(username.value)) {
                register(data, 'Username already exists.');
                return;
            }
            if (username.value.includes('@')) {
                register(data, 'That looks like an email address. Please use a username.');
                return;
            }
            email_prompt()
        }
        username.oninput = async () => {
            if (username.value === "") {
                button.enabled = false;
            } else {
                button.enabled = true;
            }
        }

        window.appendChild(title);
        window.appendChild(paragraph);
        window.appendChild(document.createElement('br'));
        window.appendChild(username);
        window.appendChild(document.createElement('br'));
        window.appendChild(document.createElement('br'));
        window.appendChild(button);

        if (error !== "") {
            const error_message = document.createElement('p');
            error_message.id = 'register-error';
            error_message.classList = 'error register-error';
            error_message.innerText = error;
            window.appendChild(error_message);
        }
    }

    username_prompt();
}

function admin(data) {
}

function e404(data) {
    set_path('/');
    const win = create_window('404-window');

    const title = document.createElement('h1');
    const paragraph = document.createElement('p');

    title.innerText = '404 - Page not found';
    title.classList = 'error-title';
    paragraph.innerText = 'The page you are looking for does not exist. Perhaps the owner of this site forgot to create it?';
    paragraph.classList = 'error-paragraph';

    // TODO: Some kind of 'perhaps you meant' thing

    win.appendChild(title);
    win.appendChild(paragraph);
}

async function load_page(path, data) {
    hide_all_windows();
    if (path === "/login" && get_cookie('username') === null) {
        login(data);
        return;
    } else if (path === "/logout") {
        logout(data);
        return;
    } else if (path === "/register") {
        register(data);
        return;
    } else if (path === "/admin" && get_cookie('user_type') == 1) {
        admin(data);
        return;
    }

    async function fetch_page(path) {
        const api = "/api/get_page";
        const send = { page: path };

        try {
            const response = await fetch(api, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(send),
            });

            if (!response.ok) {
                throw new Error('Network response was not ok');
            }

            const json = await response.json();

            if (json.error_str) {
                console.error('Error from server:', json.error_str);
                return undefined;
            }

            if (json.output_content === undefined) {
                console.error('output_content is undefined');
                return undefined;
            }

            if (json.output_content_type !== 'html') {
                throw new Error('Invalid content type: ' + json.output_content_type);
            }

            return json.output_content;
        } catch (error) {
            console.error('Fetch error:', error);
            return undefined;
        }
    }

    const content = await fetch_page(path);
    if (content !== undefined && content !== null) {
        const div = document.getElementById('content');
        div.innerHTML += content;
        return;
    } else {
        // no?
        // 404
        return e404(data);
    }
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
        if (get_cookie('user_type') == 1) {
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
            a.onclick = (event) => {
                set_path(link.path);
                load_page(link.path, data);
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

    const meta_charset = document.createElement('meta');
    meta_charset.setAttribute('charset', 'UTF-8');
    document.head.appendChild(meta_charset);

    if (data.default_site.enable_opengraph !== undefined && data.default_site.enable_opengraph) {
        // Open Graph
        const meta_og_title = document.createElement('meta');
        meta_og_title.property = 'og:title';
        meta_og_title.content = data.default_site.title;
        document.head.appendChild(meta_og_title);

        const meta_og_description = document.createElement('meta');
        meta_og_description.property = 'og:description';
        meta_og_description.content = data.default_site.description;
        document.head.appendChild(meta_og_description);

        const meta_og_type = document.createElement('meta');
        meta_og_type.property = 'og:type';
        meta_og_type.content = 'website';
        document.head.appendChild(meta_og_type);

        const meta_og_url = document.createElement('meta');
        meta_og_url.property = 'og:url';
        meta_og_url.content = window.location.href;
        document.head.appendChild(meta_og_url);
    }

    const link_favicon = document.createElement('link');
    link_favicon.rel = 'icon';
    link_favicon.href = data.default_site.favicon;
    document.head.appendChild(link_favicon);

    const css = document.createElement('link');
    css.rel = 'stylesheet';
    css.href = '/css/main.css';
    document.head.appendChild(css);
}

window.addEventListener('load', function() {
    const footer = document.querySelector('footer');
    const body = document.body;
    const nav_bar = document.querySelector('.top-bar');

    if (document.body.scrollHeight <= window.innerHeight && footer) {
        footer.classList.add('visible');
    }

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
});

function main(data) {
    set_defaults(data);
    append_footer(data);
    append_header(data);

    const div = document.createElement('div');
    div.classList = 'content';
    div.id = 'content';

    document.body.appendChild(div);

    load_page(get_path(), data);
}

document.addEventListener('DOMContentLoaded', function() {
    include("https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/highlight.min.js");
    include("https://kit.fontawesome.com/aa55cd1c33.js");

    get_site_settings().then((data) => {
        main(data);
    });
})